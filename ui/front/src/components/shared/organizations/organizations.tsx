import {
  BeamButton, Nav, Search
} from '@components/shared';
import { useMemo } from 'react';
import { Modal, Typography } from 'antd';
import { useOrganization } from '@libs/hooks/container/organization';
import styles from './organizations.module.scss';
import { OrgList } from './list';

const { Title } = Typography;

function Organizations() {
  const {
    items, searchText, type, path, pkey, isModal, showModal, closeModal, setInputText
  } = useOrganization();

  const navItems = [
    {
      key: 'all',
      to: `${path}organizations/all/1`,
      text: 'All Organizations'
    },
    {
      key: 'my',
      to: `${path}organizations/my/1`,
      text: 'My Organizations'
    }
  ];

  const repoManager = useMemo(() => (
    <div className={styles.repoHeader}>
      {pkey && <Nav type={type} items={navItems} />}

      <div className={styles.manage}>
        <div className={styles.searchWrapper}>
          <Search
            text={searchText}
            setInputText={setInputText}
            placeholder="Search by organization name or ID"
          />
        </div>
        {pkey && (
          <div className={styles.buttonWrapper}>
            <BeamButton callback={showModal}>
              Add new
            </BeamButton>
          </div>
        )}
      </div>

      <Modal
        visible={isModal}
        onCancel={closeModal}
        closable={false}
      />
    </div>
  ), [searchText, pkey]);

  return (
    <div className={styles.content}>
      <Title level={3}>Organizations</Title>
      {repoManager}
      <OrgList
        items={items}
        searchText={searchText}
        path={path}
        type={type}
        page={0}
      />
    </div>
  );
}

export default Organizations;
